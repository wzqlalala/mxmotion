# Keep the legacy MotionEngine configure isolated from the Qt application.
function(mx_import_motionengine)
    set(source "${PROJECT_SOURCE_DIR}/motionengine")
    if(MX_MOTIONENGINE_SOURCE_DIR)
        get_filename_component(source "${MX_MOTIONENGINE_SOURCE_DIR}" ABSOLUTE)
    endif()
    if(NOT EXISTS "${source}/src/motion/physics/SystemNSC.h" OR
       NOT EXISTS "${source}/packages/eigen/Eigen/Core")
        message(FATAL_ERROR "Initialize the motionengine submodule: git submodule update --init --recursive")
    endif()
    if(WIN32)
        set(engine_library "bin/motionengine.dll")
        set(modal_library "bin/MotionEngine_modal.dll")
        set(engine_byproducts "bin/motionengine.dll;lib/motionengine.lib;bin/MotionEngine_modal.dll;lib/MotionEngine_modal.lib")
    else()
        set(engine_library "lib/${CMAKE_SHARED_LIBRARY_PREFIX}motionengine${CMAKE_SHARED_LIBRARY_SUFFIX}")
        set(modal_library "lib/${CMAKE_SHARED_LIBRARY_PREFIX}MotionEngine_modal${CMAKE_SHARED_LIBRARY_SUFFIX}")
        set(engine_byproducts "${engine_library};${modal_library}")
    endif()
    if(NOT WIN32 AND NOT MX_MOTIONENGINE_SOURCE_DIR)
        # Do not alter the pinned submodule: apply compile portability fixes
        # to an isolated build-tree copy, with exact replacements checked.
        set(original_source "${source}")
        set(source "${PROJECT_BINARY_DIR}/motionengine-source")
        execute_process(COMMAND "${CMAKE_COMMAND}"
            "-DMX_ENGINE_SOURCE=${original_source}" "-DMX_ENGINE_DESTINATION=${source}"
            -P "${PROJECT_SOURCE_DIR}/cmake/PrepareMotionEngine.cmake"
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
    if(MX_MOTIONENGINE_BUILD_DIR)
        get_filename_component(build "${MX_MOTIONENGINE_BUILD_DIR}" ABSOLUTE)
        foreach(required IN LISTS engine_byproducts)
            if(NOT EXISTS "${build}/${required}")
                message(FATAL_ERROR "Missing MotionEngine build artifact: ${build}/${required}")
            endif()
        endforeach()
        foreach(required IN ITEMS motion/Config.h motion/Version.h)
            if(NOT EXISTS "${build}/${required}")
                message(FATAL_ERROR "Missing MotionEngine build artifact: ${build}/${required}")
            endif()
        endforeach()
    else()
        include(ExternalProject)
        set(build "${PROJECT_BINARY_DIR}/motionengine-build")
        file(MAKE_DIRECTORY "${build}")
        set(byproducts)
        foreach(artifact IN LISTS engine_byproducts)
            list(APPEND byproducts "${build}/${artifact}")
        endforeach()
        ExternalProject_Add(MotionEngineBuild
            SOURCE_DIR "${source}"
            BINARY_DIR "${build}"
            CMAKE_GENERATOR Ninja
            CMAKE_ARGS
                -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
                -DCMAKE_BUILD_TYPE:STRING=Release
                -DBUILD_TESTING:BOOL=OFF -DBUILD_DEMOS:BOOL=OFF -DBUILD_BENCHMARKING:BOOL=OFF
                -DENABLE_MODULE_MODAL:BOOL=ON -DENABLE_HDF5:BOOL=OFF
                -DUSE_SIMD:BOOL=OFF -DENABLE_OPENMP:BOOL=OFF -DUSE_EIGEN_OPENMP:BOOL=OFF -DENABLE_TBB:BOOL=OFF
                -DCMAKE_DISABLE_FIND_PACKAGE_Eigen3:BOOL=ON -DCMAKE_DISABLE_FIND_PACKAGE_OpenMP:BOOL=ON
                -DCMAKE_DISABLE_FIND_PACKAGE_TBB:BOOL=ON -DCMAKE_DISABLE_FIND_PACKAGE_MPI:BOOL=ON
                -DCMAKE_DISABLE_FIND_PACKAGE_Thrust:BOOL=ON
            BUILD_COMMAND "${CMAKE_COMMAND}" --build <BINARY_DIR> --target motionengine MotionEngine_modal --parallel ${MX_ENGINE_BUILD_JOBS}
            INSTALL_COMMAND ""
            BUILD_ALWAYS ON
            BUILD_BYPRODUCTS ${byproducts})
    endif()

    add_library(MotionEngine::Core SHARED IMPORTED GLOBAL)
    set_target_properties(MotionEngine::Core PROPERTIES
        IMPORTED_LOCATION "${build}/${engine_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${source}/src;${build};${source}/packages/eigen"
        INTERFACE_COMPILE_DEFINITIONS "NOMINMAX;_ENABLE_EXTENDED_ALIGNED_STORAGE;EIGEN_DONT_PARALLELIZE")
    file(STRINGS "${source}/src/motion/utils/ParserAdams.h" body_drive_declaration REGEX "enum class BodyDriveMode")
    if(body_drive_declaration)
        target_compile_definitions(MotionEngine::Core INTERFACE MX_HAS_BODY_DRIVES=1)
    endif()
    if(MSVC)
        set_target_properties(MotionEngine::Core PROPERTIES
            IMPORTED_IMPLIB "${build}/lib/motionengine.lib"
            INTERFACE_COMPILE_OPTIONS "/Zc:__cplusplus")
    endif()
    add_library(MotionEngine::Modal SHARED IMPORTED GLOBAL)
    set_target_properties(MotionEngine::Modal PROPERTIES
        IMPORTED_LOCATION "${build}/${modal_library}"
        INTERFACE_LINK_LIBRARIES MotionEngine::Core)
    if(MSVC)
        set_target_properties(MotionEngine::Modal PROPERTIES
            IMPORTED_IMPLIB "${build}/lib/MotionEngine_modal.lib")
    endif()
    if(TARGET MotionEngineBuild)
        add_dependencies(MotionEngine::Core MotionEngineBuild)
        add_dependencies(MotionEngine::Modal MotionEngineBuild)
    endif()
endfunction()

function(mx_import_qwt)
    set(prefix "${MX_QWT_ROOT}")
    if(WIN32)
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
    else()
        find_path(qwt_include NAMES qwt_plot.h HINTS "${prefix}/include" PATH_SUFFIXES qwt qwt-qt5 NO_CACHE REQUIRED)
        find_library(qwt_lib NAMES qwt-qt5 qwt qwt6 HINTS "${prefix}/lib" NO_CACHE REQUIRED)
        set(qwt_dll "${qwt_lib}")
    endif()
    add_library(Qwt::Qwt SHARED IMPORTED GLOBAL)
    set_target_properties(Qwt::Qwt PROPERTIES
        IMPORTED_LOCATION "${qwt_dll}"
        INTERFACE_INCLUDE_DIRECTORIES "${qwt_include}"
        INTERFACE_LINK_LIBRARIES Qt5::Widgets)
    if(WIN32)
        set_target_properties(Qwt::Qwt PROPERTIES
            IMPORTED_IMPLIB "${qwt_lib}"
            INTERFACE_COMPILE_DEFINITIONS QWT_DLL)
    endif()
endfunction()
