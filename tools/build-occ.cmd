@echo off
setlocal
pushd "%~dp0.." || exit /b 1

rem Use an x64 Native Tools prompt, or pass the path to vcvars64.bat.
if defined MX_VCVARS64 (
    call "%MX_VCVARS64%"
    if errorlevel 1 goto :failed
)
where cl.exe >nul 2>&1 || (
    >&2 echo MSVC not active. Open an x64 Native Tools prompt or set MX_VCVARS64.
    goto :failed
)
if not defined MX_QT_ROOT (
    >&2 echo Set MX_QT_ROOT to the Qt 5.15 MSVC x64 prefix.
    goto :failed
)
if not defined MX_OCCT_ROOT (
    >&2 echo Set MX_OCCT_ROOT to the OCCT SDK Library directory.
    goto :failed
)
if not defined MX_NETGEN_ROOT (
    >&2 echo Set MX_NETGEN_ROOT to the OCC-enabled Netgen SDK prefix.
    goto :failed
)
if not defined MX_BUILD_DIR set "MX_BUILD_DIR=build-occ"
if not defined MX_BUILD_JOBS set "MX_BUILD_JOBS=4"
if not defined MX_TEST_DESKTOP set "MX_TEST_DESKTOP=OFF"
if not defined MX_ENGINE_BUILD_JOBS set "MX_ENGINE_BUILD_JOBS=%MX_BUILD_JOBS%"
if not defined MX_QWT_ROOT set "MX_QWT_ROOT=%CD%\.local\qwt-sdk"
if not exist "%MX_QWT_ROOT%\include\qwt_plot.h" (
    cmake -P tools/build-qwt.cmake
    if errorlevel 1 goto :failed
)

cmake -S . -B "%MX_BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DMX_TEST_DESKTOP=%MX_TEST_DESKTOP%" ^
    "-DCMAKE_PREFIX_PATH=%MX_QT_ROOT%" "-DMX_OCCT_ROOT=%MX_OCCT_ROOT%" ^
    "-DMX_NETGEN_ROOT=%MX_NETGEN_ROOT%" "-DMX_QTADS_SOURCE_DIR=%MX_QTADS_SOURCE_DIR%" ^
    "-DMX_QWT_ROOT=%MX_QWT_ROOT%" "-DMX_MOTIONENGINE_BUILD_DIR=%MX_MOTIONENGINE_BUILD_DIR%" ^
    "-DMX_MOTIONENGINE_SOURCE_DIR=%MX_MOTIONENGINE_SOURCE_DIR%" ^
    "-DMX_ENGINE_BUILD_JOBS=%MX_ENGINE_BUILD_JOBS%"
if errorlevel 1 goto :failed
cmake --build "%MX_BUILD_DIR%" --parallel %MX_BUILD_JOBS%
if errorlevel 1 goto :failed
ctest --test-dir "%MX_BUILD_DIR%" --output-on-failure
if errorlevel 1 goto :failed
for %%I in ("%MX_BUILD_DIR%\bin\MxMotion.exe") do echo MxMotion: %%~fI
popd
exit /b 0

:failed
popd
exit /b 1
