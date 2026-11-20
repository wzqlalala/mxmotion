#!/usr/bin/env bash
# Linux build of the same OCC / MotionEngine Qt application.
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${MX_BUILD_DIR:-${repo_dir}/build-occ}"
build_jobs="${MX_BUILD_JOBS:-4}"
desktop_tests="${MX_TEST_DESKTOP:-OFF}"
cmake -S "${repo_dir}" -B "${build_dir}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DMX_ENGINE_BUILD_JOBS="${build_jobs}" \
    -DMX_TEST_DESKTOP="${desktop_tests}" "$@"
# Read the configured value after any extra -D arguments have overridden the
# environment default. Match CMake's false constants; other values are enabled.
desktop_tests="$(sed -n 's/^MX_TEST_DESKTOP:[^=]*=//p' "${build_dir}/CMakeCache.txt")"
case "${desktop_tests^^}" in
    ""|0|OFF|NO|FALSE|N|IGNORE|NOTFOUND|*-NOTFOUND) desktop_tests=OFF ;;
    *) desktop_tests=ON ;;
esac
cmake --build "${build_dir}" --parallel "${build_jobs}"
if [[ "${desktop_tests^^}" == ON && -z "${DISPLAY:-}" ]]; then
    LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a ctest --test-dir "${build_dir}" --output-on-failure
else
    ctest --test-dir "${build_dir}" --output-on-failure
fi
