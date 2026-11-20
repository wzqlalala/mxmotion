@echo off
setlocal EnableDelayedExpansion

REM 调试启动脚本：用 Visual Studio 的 /debugexe 打开 MxMech 批量 CLI
REM 自动查找常见 VS 路径；如果找不到，请手动修改下面的 VS_PATH。

REM 脚本位于 tools\，项目根目录是 tools\..
set "PROJECT_ROOT=%~dp0.."
set "MXMECH_EXE=%PROJECT_ROOT%\build\Release\MxMech.exe"
set "BATCH_CONFIG=%PROJECT_ROOT%\tools\batch_cli_example.json"

REM 加载所有依赖 DLL 的构建目录，避免 STATUS_DLL_NOT_FOUND
set "DEP_PATHS=%PROJECT_ROOT%\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\ThirdLib\install\bin"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Basic\RebuildBasicData\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Basic\RebuildModelData\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Basic\RebuildMesh\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Software\Common\MxFunctions\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Software\Common\MXGeoCommand\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Software\Common\PlotView\build\Release"
set "DEP_PATHS=%DEP_PATHS%;%PROJECT_ROOT%\..\..\..\Software\Common\Render\build\Release"
set "PATH=%DEP_PATHS%;%PATH%"

set "FOUND_VS="
for %%p in (
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\devenv.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\devenv.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\IDE\devenv.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\devenv.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\devenv.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\Common7\IDE\devenv.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\devenv.exe"
) do (
    if exist %%p (
        set "FOUND_VS=%%p"
        goto :run
    )
)

:run
if defined FOUND_VS (
    echo Found VS: %FOUND_VS%
    echo Starting debug with batch config: %BATCH_CONFIG%
    "%FOUND_VS%" /debugexe "%MXMECH_EXE%" --headless --batch "%BATCH_CONFIG%"
) else (
    echo ERROR: Could not find devenv.exe in common paths.
    echo Please set VS_PATH manually in this script and try again.
)
endlocal
pause
