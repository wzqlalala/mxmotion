# Build-only compatibility for the legacy and current pinned engine revisions.
# Copying preserves every source and keeps the git submodule byte-for-byte clean.
if(NOT IS_DIRECTORY "${MX_ENGINE_SOURCE}/src" OR NOT MX_ENGINE_DESTINATION)
    message(FATAL_ERROR "Expected MotionEngine source and build-tree destination")
endif()
file(MAKE_DIRECTORY "${MX_ENGINE_DESTINATION}")
file(COPY "${MX_ENGINE_SOURCE}/" DESTINATION "${MX_ENGINE_DESTINATION}"
    PATTERN ".git" EXCLUDE PATTERN "build" EXCLUDE
    PATTERN "src/CMakeLists.txt" EXCLUDE PATTERN "src/motion/core/Timer.h" EXCLUDE
    PATTERN "src/motion/utils/ParserAdams.cpp" EXCLUDE)
set(path "${MX_ENGINE_DESTINATION}/src/CMakeLists.txt")
file(READ "${MX_ENGINE_SOURCE}/src/CMakeLists.txt" content)
set(old "set(MOTION_CXX_FLAGS \"\${CMAKE_CXX_FLAGS} /utf-8\")")
set(new "set(MOTION_CXX_FLAGS \"\${CMAKE_CXX_FLAGS}\")\nif(MSVC)\n  set(MOTION_CXX_FLAGS \"\${MOTION_CXX_FLAGS} /utf-8\")\nendif()")
string(FIND "${content}" "${old}" found)
if(found EQUAL -1)
    set(guarded "if(MSVC)\n  set(MOTION_CXX_FLAGS \"\${MOTION_CXX_FLAGS} /utf-8\")\nendif()")
    string(FIND "${content}" "${guarded}" guarded_found)
    if(guarded_found EQUAL -1)
        message(FATAL_ERROR "MotionEngine compiler flags are neither the legacy nor the reviewed portable form; review the submodule revision")
    endif()
else()
    string(REPLACE "${old}" "${new}" content "${content}")
endif()
file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" @ONLY)

# Earlier engine revisions renamed std::chrono accidentally; current revisions
# already use the correct namespace. Initialize the accumulator in either case.
set(path "${MX_ENGINE_DESTINATION}/src/motion/core/Timer.h")
file(READ "${MX_ENGINE_SOURCE}/src/motion/core/Timer.h" content)
string(REPLACE "std::motion::" "std::chrono::" content "${content}")
string(REPLACE "MotionTimer() {}" "MotionTimer() : m_total(seconds_type(0)) {}" content "${content}")
file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" @ONLY)

# This numeric parser is instantiated with int/double; both conversion branches
# are valid in C++14. Preserve the engine's declared language standard/ABI.
set(path "${MX_ENGINE_DESTINATION}/src/motion/utils/ParserAdams.cpp")
file(READ "${MX_ENGINE_SOURCE}/src/motion/utils/ParserAdams.cpp" content)
set(old "if constexpr (std::is_same_v<T, int>)")
set(new "if (std::is_same<T, int>::value)")
string(FIND "${content}" "${old}" found)
if(found EQUAL -1)
    string(FIND "${content}" "${new}" portable_found)
    if(portable_found EQUAL -1)
        message(FATAL_ERROR "MotionEngine numeric parser is neither the legacy nor the reviewed C++14 form; review the submodule revision")
    endif()
else()
    string(REPLACE "${old}" "${new}" content "${content}")
endif()
file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" @ONLY)
