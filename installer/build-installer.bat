@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%..\out\build\x64-Release"
set "STAGE_DIR=%SCRIPT_DIR%staging"

echo [1/3] Installing built files to staging area...
if exist "%STAGE_DIR%" rd /s /q "%STAGE_DIR%"
cmake --install "%BUILD_DIR%" --prefix "%STAGE_DIR%" --config RelWithDebInfo
if errorlevel 1 (
    echo ERROR: cmake --install failed.
    exit /b 1
)

echo [2/3] Building WiX objects...
candle "%SCRIPT_DIR%PTZCommander.wxs" -dSourceDir="%STAGE_DIR%" -out "%SCRIPT_DIR%PTZCommander.wixobj"
if errorlevel 1 (
    echo ERROR: candle failed.
    exit /b 1
)

echo [3/3] Linking MSI...
light "%SCRIPT_DIR%PTZCommander.wixobj" -out "%SCRIPT_DIR%PTZCommander.msi" -ext WixUIExtension
if errorlevel 1 (
    echo ERROR: light failed.
    exit /b 1
)

echo.
echo SUCCESS: PTZCommander.msi created.
echo You can clean up the 'staging' folder manually if desired.

endlocal
