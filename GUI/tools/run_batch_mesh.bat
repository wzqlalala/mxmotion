@echo off
setlocal EnableDelayedExpansion

REM MxMech batch mesh CLI launcher
REM Auto-sets PATH from known mech project runtime directories

REM === Qt5 runtime ===
set "QT_DIR=D:\Qt\Qt5.13.2\5.13.2\msvc2017_64"
set "PATH=%QT_DIR%\bin;%PATH%"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%QT_DIR%\plugins\platforms"
set "QT_QPA_PLATFORM=windows"

REM === OpenCASCADE 7.9.0 runtime DLLs ===
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildMesh\bin\_deps\megrid-src\Me3rd\occ7_9_0\bin;%PATH%"

REM === Core mech module runtime DLLs ===
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildGeometry\bin\Release;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildGeometry\install\bin;%PATH%"

set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildModelData\bin\Release;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildModelData\install\bin;%PATH%"

set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildBasicData\bin\Release;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildBasicData\install\bin;%PATH%"

set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildMesh\bin\Release;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildMesh\bin\Release\bin;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildMesh\install\bin;%PATH%"

REM === PlotView / Render runtime DLLs ===
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\PlotView\bin\Release;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\PlotView\install\bin;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildRender\bin\Release;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Basic\RebuildRender\install\bin;%PATH%"

REM === freetype runtime DLL ===
set "PATH=F:\2026\dev\MxSim.MechanicalV3.0.0_Build015_0930\Version20260815-RebuildMech\tools\freetype;%PATH%"
set "PATH=C:\Users\MX83\.conan2\p\freet006a3debe4d47\p\bin;%PATH%"

REM === Python 3.7 runtime ===
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Software\Common\RebuildGUI\bin\Release\python;%PATH%"
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\Software\Common\RebuildGUI\bin\Release\python\bin;%PATH%"

REM === OpenSSL 3 runtime ===
set "PATH=C:\Users\MX83\.conan2\p\opens8fdafcb2bafc8\p\bin;%PATH%"
set "PATH=C:\Users\MX83\.conan2\p\opens98c6739d868ea\p\bin;%PATH%"

REM === TBB runtime ===
set "PATH=C:\Users\MX83\.conan2\p\onetb2c0569e1fcc1b\p\bin;%PATH%"

REM === OpenSceneGraph runtime ===
set "PATH=C:\Users\MX83\.conan2\p\opens3dcdb024a1a5e\p\bin;%PATH%"

REM === Auto-add all Conan2 package bin directories ===
for /r "C:\Users\MX83\.conan2\p" %%d in (.) do (
    if /i "%%~nd"=="bin" (
        set "PATH=%%~fd;!PATH!"
    )
)

REM === Auto-add all ThirdLib conan_install bin directories ===
for /r "F:\2026\ProjectScript\RebuildMech\mech\ThirdLib\conan_install" %%d in (.) do (
    if /i "%%~nd"=="bin" (
        set "PATH=%%~fd;!PATH!"
    )
)

REM === Packaged launcher runtime (contains qtads, qwt, ffmpeg, Qt5, OCC, etc.) ===
set "LAUNCHER_DIR=F:\2026\dev\MxSim.MechanicalV3.0.0_Build015_0930\Version20260815-RebuildMech"
if exist "%LAUNCHER_DIR%" (
    set "PATH=%LAUNCHER_DIR%;%PATH%"
    set "PATH=%LAUNCHER_DIR%\Application\ffmpeg;%PATH%"
    set "PATH=%LAUNCHER_DIR%\tools\qtads;%PATH%"
    set "PATH=%LAUNCHER_DIR%\tools\qwt;%PATH%"
)

REM === Conan2 cache runtime DLLs ===
set "PATH=C:\Users\MX83\.conan2\p\qtadse7d8b9c2e9b58\p\bin;%PATH%"
set "PATH=C:\Users\MX83\.conan2\p\qwtf8d11a5565765\p\bin;%PATH%"
set "PATH=C:\Users\MX83\.conan2\p\ffmpe39a1edbea104a\p\bin;%PATH%"
set "PATH=C:\Users\MX83\.conan2\p\ffmpea2efd7eb5cd54\p\bin;%PATH%"

REM === ThirdLib runtime ===
set "PATH=F:\2026\ProjectScript\RebuildMech\mech\ThirdLib\conan_install;%PATH%"

REM Target executable and config
set "MXMECH_EXE=F:\2026\ProjectScript\RebuildMech\mech\Software\Common\RebuildGUI\bin\Release\MxMech.exe"
set "BATCH_CONFIG=F:\2026\ProjectScript\RebuildMech\mech\Software\Common\RebuildGUI\tools\batch_cli_example.json"
set "LOG_FILE=F:\2026\ProjectScript\RebuildMech\mech\Software\Common\RebuildGUI\tools\batch.log"

echo Starting MxMech batch mesh CLI
echo EXE: %MXMECH_EXE%
echo CONFIG: %BATCH_CONFIG%
echo LOG: %LOG_FILE%
echo.

if not exist "%MXMECH_EXE%" (
    echo ERROR: MxMech.exe not found at %MXMECH_EXE%
    goto :end
)

if not exist "%BATCH_CONFIG%" (
    echo ERROR: batch config not found at %BATCH_CONFIG%
    goto :end
)

"%MXMECH_EXE%" --batch "%BATCH_CONFIG%"

:end
endlocal
