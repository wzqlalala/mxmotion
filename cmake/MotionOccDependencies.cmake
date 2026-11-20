# The pinned MotionEngine project uses its own top-level variables and exports.
# Keep its configure isolated, and consume its Release build tree rather than
# loading the legacy SDK package config (which is not relocatable at this pin).
function(mx_import_motionengine)
    set(source "${PROJECT_SOURCE_DIR}/motionengine")
    if(NOT EXISTS "${source}/src/motion/physics/SystemNSC.h" OR
       NOT EXISTS "${source}/packages/eigen/Eigen/Core")
        message(FATAL_ERROR "Initialize the motionengine submodule: git submodule update --init --recursive")
    endif()
    if(MX_MOTIONENGINE_BUILD_DIR)
        get_filename_component(build "${MX_MOTIONENGINE_BUILD_DIR}" ABSOLUTE)
        foreach(required IN ITEMS bin/motionengine.dll lib/motionengine.lib motion/Config.h motion/Version.h)
            if(NOT EXISTS "${build}/${required}")
                message(FATAL_ERROR "Missing MotionEngine build artifact: ${build}/${required}")
            endif()
        endforeach()
    else()
        include(ExternalProject)
        set(build "${PROJECT_BINARY_DIR}/motionengine-build")
        file(MAKE_DIRECTORY "${build}")
        ExternalProject_Add(MotionEngineBuild
            SOURCE_DIR "${source}"
            BINARY_DIR "${build}"
            CMAKE_GENERATOR Ninja
            CMAKE_ARGS
                -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                -DCMAKE_BUILD_TYPE:STRING=Release
                -DBUILD_TESTING:BOOL=OFF -DBUILD_DEMOS:BOOL=OFF -DBUILD_BENCHMARKING:BOOL=OFF
                -DENABLE_MODULE_MODAL:BOOL=OFF -DENABLE_HDF5:BOOL=OFF
                -DUSE_SIMD:BOOL=OFF -DENABLE_OPENMP:BOOL=OFF -DUSE_EIGEN_OPENMP:BOOL=OFF -DENABLE_TBB:BOOL=OFF
                -DCMAKE_DISABLE_FIND_PACKAGE_Eigen3:BOOL=ON -DCMAKE_DISABLE_FIND_PACKAGE_OpenMP:BOOL=ON
                -DCMAKE_DISABLE_FIND_PACKAGE_TBB:BOOL=ON -DCMAKE_DISABLE_FIND_PACKAGE_MPI:BOOL=ON
                -DCMAKE_DISABLE_FIND_PACKAGE_Thrust:BOOL=ON
            BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --target motionengine --parallel ${MX_ENGINE_BUILD_JOBS}
            # The pin's stock install also references an absent chpf directory.
            INSTALL_COMMAND ""
            BUILD_ALWAYS ON
            BUILD_BYPRODUCTS "${build}/bin/motionengine.dll" "${build}/lib/motionengine.lib")
    endif()

    add_library(MotionEngine::Core SHARED IMPORTED GLOBAL)
    set_target_properties(MotionEngine::Core PROPERTIES
        IMPORTED_IMPLIB "${build}/lib/motionengine.lib"
        IMPORTED_LOCATION "${build}/bin/motionengine.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${source}/src;${build};${source}/packages/eigen"
        INTERFACE_COMPILE_DEFINITIONS "NOMINMAX;_ENABLE_EXTENDED_ALIGNED_STORAGE;EIGEN_DONT_PARALLELIZE"
        INTERFACE_COMPILE_OPTIONS "/Zc:__cplusplus")
    if(TARGET MotionEngineBuild)
        add_dependencies(MotionEngine::Core MotionEngineBuild)
    endif()
endfunction()

function(mx_import_qwt)
    set(prefix "${MX_QWT_ROOT}")
    find_path(qwt_include NAMES qwt_plot.h PATHS "${prefix}/include" NO_DEFAULT_PATH NO_CACHE)
    find_library(qwt_lib NAMES qwt qwt6 PATHS "${prefix}/lib" NO_DEFAULT_PATH NO_CACHE)
    find_file(qwt_dll NAMES qwt.dll qwt6.dll PATHS "${prefix}/bin" "${prefix}/lib" NO_DEFAULT_PATH NO_CACHE)
    if(NOT qwt_include OR NOT qwt_lib OR NOT qwt_dll)
        message(FATAL_ERROR "Set MX_QWT_ROOT to the Qt 5.15 x64 Qwt 6.2.0 SDK, or run cmake -P tools/build-qwt.cmake with MX_QT_ROOT set.")
    endif()
    file(STRINGS "${qwt_include}/qwt_global.h" version_line REGEX "^#define[ \t]+QWT_VERSION_STR[ \t]+\"6\\.2\\.0\"")
    if(NOT version_line)
        message(FATAL_ERROR "Qwt 6.2.0 is required: ${qwt_include}/qwt_global.h")
    endif()
    add_library(Qwt::Qwt SHARED IMPORTED GLOBAL)
    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_IMPLIB "${qwt_lib}"
        IMPORTED_LOCATION "${qwt_dll}"
        INTERFACE_INCLUDE_DIRECTORIES "${qwt_include}"
        INTERFACE_COMPILE_DEFINITIONS QWT_DLL
        INTERFACE_LINK_LIBRARIES Qt5::Widgets)
endfunction()
