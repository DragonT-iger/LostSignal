@echo off
chcp 65001 >nul
setlocal

set "PROJECT_DIR=%~dp0"
set "SCRIPT_FILE=%PROJECT_DIR%tools\PIE_ESC_to_ShiftESC.ps1"

echo =====================================================
echo   UE 5.7: PIE stop key ESC -^> Shift+ESC (permanent)
echo =====================================================
echo.

if not exist "%SCRIPT_FILE%" (
    echo [ERROR] PowerShell helper was not found:
    echo         %SCRIPT_FILE%
    echo.
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_FILE%"
set "RESULT=%ERRORLEVEL%"

echo.
if not "%RESULT%"=="0" (
    echo Failed. Exit code: %RESULT%
    pause
    exit /b %RESULT%
)

pause
