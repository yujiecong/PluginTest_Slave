@echo off
setlocal

set UE_EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT_DIR=c:\Users\42458\Documents\Unreal Projects\PluginTest
set PROJECT=%PROJECT_DIR%\PluginTest.uproject
set TEST_SCRIPT=%PROJECT_DIR%\Scripts\run_full_pipeline_test.py
set OUTPUT_DIR=%PROJECT_DIR%\Saved\UEMotionTest\full_pipeline

echo ========================================
echo   UEMotion Smoke Test Launcher
echo ========================================
echo.

if not exist "%UE_EDITOR%" (
    echo [ERROR] UnrealEditor.exe not found at: %UE_EDITOR%
    exit /b 1
)

if not exist "%TEST_SCRIPT%" (
    echo [ERROR] Test script not found at: %TEST_SCRIPT%
    exit /b 1
)

if exist "%OUTPUT_DIR%" (
    echo [CLEAN] Removing previous output...
    rmdir /s /q "%OUTPUT_DIR%"
)

echo [LAUNCH] Starting UE5 Editor with test script...
echo   Project : %PROJECT%
echo   Script  : %TEST_SCRIPT%
echo.

"%UE_EDITOR%" "%PROJECT%" -ExecCmds="py %TEST_SCRIPT%" -NoSound -Unattended -NoSplash -Test

set EXIT_CODE=%ERRORLEVEL%

echo.
echo ========================================
if %EXIT_CODE% EQU 0 (
    echo   RESULT: PASSED (exit code 0)
) else (
    echo   RESULT: FAILED (exit code %EXIT_CODE%)
)
echo ========================================

endlocal
exit /b %EXIT_CODE%
