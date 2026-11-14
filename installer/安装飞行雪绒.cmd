@echo off
chcp 65001 >nul
setlocal
set "SCRIPT=%~dp0install-online.ps1"
if not exist "%SCRIPT%" (
    echo 找不到 install-online.ps1，请确保它与本文件在同一目录。
    pause
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
echo.
pause
