cmake_minimum_required(VERSION 3.21)
project(MxMotionOcc VERSION 1.0 LANGUAGES CXX)

# The original, full SDK builds remain in GUI/ and ModelData/.
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The OCC profile requires Windows and MSVC x64.")
endif()
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Release SDK configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "Use Release with the OCCT and Netgen SDKs.")
endif()
enable_language(RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
include(CTest)
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui)
option(MX_BUILD_GUI "Build the MxMotion desktop application" ON)
option(MX_TEST_DESKTOP "Exercise the real Qt/ADS and OpenGL window (requires an interactive desktop)" OFF)

set(MX_OCCT_ROOT "" CACHE PATH "OCCT 7.8.x x64 SDK (include/opencascade, lib, bin)")
set(MX_NETGEN_ROOT "" CACHE PATH "Netgen 6.2.2505 x64 SDK, built with OCC")
set(MX_QTADS_SOURCE_DIR "" CACHE PATH "Optional local checkout of Qt Advanced Docking System 4.4.1")
set(MX_QWT_ROOT "${PROJECT_SOURCE_DIR}/.local/qwt-sdk" CACHE PATH "Qwt 6.2.0 Qt 5.15 MSVC x64 SDK")
set(MX_MOTIONENGINE_BUILD_DIR "" CACHE PATH "Optional matching Release Ninja build of the pinned motionengine")
set(MX_ENGINE_BUILD_JOBS "4" CACHE STRING "Parallel jobs for the isolated MotionEngine build")
include(cmake/MotionOccDependencies.cmake)
option(MX_DEPLOY_QT "Deploy the Qt runtime beside MxMotion.exe" ON)

if(NOT EXISTS "${MX_OCCT_ROOT}/include/opencascade/STEPControl_Reader.hxx" OR
   NOT EXISTS "${MX_OCCT_ROOT}/bin/TKernel.dll")
    message(FATAL_ERROR "Set MX_OCCT_ROOT to the OCCT SDK's Library directory.")
endif()
if(NOT EXISTS "${MX_NETGEN_ROOT}/include/nglib_occ.h" OR
   NOT EXISTS "${MX_NETGEN_ROOT}/bin/nglib.dll")
    message(FATAL_ERROR "Set MX_NETGEN_ROOT to an OCC-enabled Netgen SDK with headers, libraries and DLLs.")
endif()

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
endfunction()

set(model_dir "${CMAKE_CURRENT_SOURCE_DIR}/ModelData/src/MotionOcc")
add_library(MotionOccData STATIC
    "${model_dir}/CadDocument.cpp"
    "${model_dir}/FemMesh.cpp")
target_include_directories(MotionOccData PUBLIC "${model_dir}")
target_include_directories(MotionOccData SYSTEM PUBLIC "${MX_OCCT_ROOT}/include/opencascade")
target_link_libraries(MotionOccData PUBLIC Qt5::Gui)
target_compile_options(MotionOccData PRIVATE /utf-8)
# Some relocated Windows OCCT exports still refer to the packager's disk.
# Link directly to the SDK's import libraries without modifying the installation.
foreach(lib IN ITEMS TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKGeomAlgo
        TKTopAlgo TKPrim TKBO TKBool TKMesh TKShHealing TKXSBase TKDE TKDESTEP TKDEIGES)
    if(NOT EXISTS "${MX_OCCT_ROOT}/lib/${lib}.lib")
        message(FATAL_ERROR "Missing OCCT import library: ${MX_OCCT_ROOT}/lib/${lib}.lib")
    endif()
    target_link_libraries(MotionOccData PRIVATE "${MX_OCCT_ROOT}/lib/${lib}.lib")
endforeach()

add_executable(MxMotionMeshWorker "${model_dir}/NetgenWorker.cpp")
target_include_directories(MxMotionMeshWorker SYSTEM PRIVATE "${MX_NETGEN_ROOT}/include")
find_library(MX_NGLIB NAMES nglib libnglib PATHS "${MX_NETGEN_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
target_link_libraries(MxMotionMeshWorker PRIVATE "${MX_NGLIB}")
target_compile_definitions(MxMotionMeshWorker PRIVATE WIN32 NOMINMAX)
target_compile_options(MxMotionMeshWorker PRIVATE /utf-8)

file(GLOB occ_dlls CONFIGURE_DEPENDS "${MX_OCCT_ROOT}/bin/*.dll")
file(GLOB netgen_dlls CONFIGURE_DEPENDS "${MX_NETGEN_ROOT}/bin/*.dll")
add_custom_target(MotionOccRuntime
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${occ_dlls} ${netgen_dlls}
            "$<TARGET_FILE:Qt5::Core>" "$<TARGET_FILE:Qt5::Gui>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    VERBATIM)
add_dependencies(MxMotionMeshWorker MotionOccRuntime)
mx_import_motionengine()
add_library(MotionOccRigid STATIC "${model_dir}/RigidMotion.cpp" "${model_dir}/RigidProject.cpp")
target_link_libraries(MotionOccRigid PUBLIC MotionOccData PRIVATE MotionEngine::Core)
target_compile_options(MotionOccRigid PRIVATE /utf-8)
target_compile_definitions(MotionOccRigid PRIVATE _HAS_STD_BYTE=0)
if(TARGET MotionEngineBuild)
    add_dependencies(MotionOccRigid MotionEngineBuild)
endif()
add_custom_target(MotionEngineRuntime
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:MotionEngine::Core>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
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
    add_executable(MotionOccBackendSmoke tests/MotionOccBackendSmoke.cpp)
    target_link_libraries(MotionOccBackendSmoke PRIVATE MotionOccData)
    target_compile_options(MotionOccBackendSmoke PRIVATE /utf-8)
    add_dependencies(MotionOccBackendSmoke MxMotionMeshWorker)
    add_test(NAME MotionOcc.CadMesh COMMAND MotionOccBackendSmoke "$<TARGET_FILE:MxMotionMeshWorker>")
    set_tests_properties(MotionOcc.CadMesh PROPERTIES TIMEOUT 120)

    add_executable(MotionOccRigidSmoke tests/MotionOccRigidSmoke.cpp)
    target_link_libraries(MotionOccRigidSmoke PRIVATE MotionOccRigid)
    target_compile_options(MotionOccRigidSmoke PRIVATE /utf-8)
    add_dependencies(MotionOccRigidSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.Rigid COMMAND MotionOccRigidSmoke)
    set_tests_properties(MotionOcc.Rigid PROPERTIES TIMEOUT 120)
endif()
