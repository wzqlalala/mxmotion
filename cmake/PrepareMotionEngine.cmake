# Build-only portability adaptation for the pinned Windows-oriented engine.
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
set(new "set(MOTION_CXX_FLAGS \"\${CMAKE_CXX_FLAGS}\")\nif(MSVC)\n  string(APPEND MOTION_CXX_FLAGS \" /utf-8\")\nendif()")
string(FIND "${content}" "${old}" found)
if(found EQUAL -1)
    message(FATAL_ERROR "MotionEngine portability patch no longer matches src/CMakeLists.txt; review the submodule revision")
endif()
string(REPLACE "${old}" "${new}" content "${content}")
file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" @ONLY)

# The engine rename accidentally changed the standard chrono namespace only in
# the non-Windows timer. Value-initialize its accumulator, as the Win32 timer does.
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
    message(FATAL_ERROR "MotionEngine C++14 parser adaptation no longer matches; review the submodule revision")
endif()
string(REPLACE "${old}" "${new}" content "${content}")
file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" @ONLY)
