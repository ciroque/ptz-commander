@echo off
setlocal
rem Build x64-Release with the VS developer environment, then launch the app without waiting for it.
call "S:\Program Files\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cmake --build "%~dp0out\build\x64-Release" || exit /b 1
start "PTZ Commander" /d "%~dp0out\build\x64-Release" "%~dp0out\build\x64-Release\ptz_commander.exe"
