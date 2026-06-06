@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%..\out\build\x64-Release"
set "INSTALLER_BUILD_DIR=%SCRIPT_DIR%build"
set "STAGE_DIR=%INSTALLER_BUILD_DIR%\staging"

echo [1/4] Cleaning previous installer build artifacts...
if exist "%INSTALLER_BUILD_DIR%" rd /s /q "%INSTALLER_BUILD_DIR%"

echo [2/4] Installing built files to staging area...
rem Ensure the staging directory is fully cleaned before the cmake --install step.
rem This prevents stale files (e.g. old ptz_commander.exe or icon resources) from
rem being left behind and picked up by the WiX build.
if exist "%STAGE_DIR%" rd /s /q "%STAGE_DIR%"
cmake --install "%BUILD_DIR%" --prefix "%STAGE_DIR%" --config RelWithDebInfo
if errorlevel 1 (
    echo ERROR: cmake --install failed.
    exit /b 1
)

rem Ensure additional files needed by the WiX build (icon for ARP/shortcuts, license RTF)
rem are present in staging. These are also covered by install() rules in CMakeLists.txt,
rem but the explicit copies make the script resilient against an out-of-date configured build tree.
copy /y "%SCRIPT_DIR%..\ptz-commander.ico" "%STAGE_DIR%\" >nul
copy /y "%SCRIPT_DIR%License.rtf" "%STAGE_DIR%\" >nul

rem Note: the executable icon is embedded at compile time via ptz-commander.rc + ptz-commander.ico.
rem For best "Extra large icons" quality, ptz-commander.ico should contain a 256x256 PNG layer
rem (see comment in CMakeLists.txt). The .ico here is mainly for the installer (shortcuts/ARP).

echo [3/4] Building WiX objects...
candle "%SCRIPT_DIR%PTZCommander.wxs" -dSourceDir="%STAGE_DIR%" -out "%INSTALLER_BUILD_DIR%\PTZCommander.wixobj"
if errorlevel 1 (
    echo ERROR: candle failed.
    exit /b 1
)

echo [4/4] Linking MSI...
light "%INSTALLER_BUILD_DIR%\PTZCommander.wixobj" -out "%INSTALLER_BUILD_DIR%\PTZCommander.msi" -ext WixUIExtension
if errorlevel 1 (
    echo ERROR: light failed.
    exit /b 1
)

echo.
echo SUCCESS: PTZCommander.msi created in %INSTALLER_BUILD_DIR%
echo You can clean up the 'build' folder manually if desired.

endlocal
