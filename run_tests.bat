@echo off
setlocal

set UEBUILD="C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat"
set UE_EDITOR="C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe"
set PROJECT_DIR=c:\Users\42458\Documents\Unreal Projects\PluginTest
set PROJECT=%PROJECT_DIR%\PluginTest.uproject
set TEST_SCRIPT=%PROJECT_DIR%/Scripts/tests/run_all_tests.py
set REPORT_FILE=%PROJECT_DIR%/Scripts/tests/test_report/report.json

echo ========================================
echo  UEMotion Build + Test Runner
echo ========================================
echo.

echo [1/3] Building PluginTestEditor...
echo.

%UEBUILD% PluginTestEditor Win64 Development "%PROJECT%" -WaitMutex -FromMsBuild

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo  BUILD FAILED (Error Code: %ERRORLEVEL%)
    echo  Skipping tests.
    echo ========================================
    endlocal
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo  BUILD SUCCEEDED
echo ========================================
echo.

if not exist %UE_EDITOR% (
    echo [ERROR] UnrealEditor.exe not found at %UE_EDITOR%
    echo         Please update UE_EDITOR path in run_tests.bat
    endlocal
    exit /b 1
)

echo [2/3] Running tests via UE5 Editor...
echo.

%UE_EDITOR% "%PROJECT%" -ExecCmds="py %TEST_SCRIPT%" -NoSound -Unattended -NoSplash

echo.
echo [3/3] Cleanup...
if exist "%PROJECT_DIR%\Plugins\UEMotionPlugin\Packaged" (
    rmdir /s /q "%PROJECT_DIR%\Plugins\UEMotionPlugin\Packaged"
    echo   Removed: Plugins\UEMotionPlugin\Packaged
) else (
    echo   Skip: Packaged directory not found
)

echo.
if exist "%REPORT_FILE%" (
    echo ========================================
    echo  Test report saved to:
    echo  %REPORT_FILE%
    echo ========================================
) else (
    echo ========================================
    echo  ERROR: No test report generated
    echo ========================================
)

endlocal
