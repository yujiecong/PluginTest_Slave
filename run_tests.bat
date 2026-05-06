@echo off
setlocal

set UE_EDITOR="C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe"
set PROJECT_DIR=c:/Users/42458/Documents/Unreal Projects/PluginTest
set PROJECT=%PROJECT_DIR%/PluginTest.uproject
set TEST_SCRIPT=%PROJECT_DIR%/Scripts/tests/run_all_tests.py

echo ========================================
echo  UEMotion Test Runner
echo ========================================
echo.

if not exist %UE_EDITOR% (
    echo [ERROR] UnrealEditor.exe not found at %UE_EDITOR%
    echo         Please update UE_EDITOR path in run_tests.bat
    exit /b 1
)

echo [1/1] Running tests via UE5 Editor...
echo.

%UE_EDITOR% %PROJECT% -ExecutePythonScript="exec(compile(open(r'%TEST_SCRIPT%').read(), r'%TEST_SCRIPT%', 'exec'), {'__file__': r'%TEST_SCRIPT%'})" -TestExit -NoSound -Unattended -NoSplash

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo  TESTS PASSED
    echo ========================================
) else (
    echo.
    echo ========================================
    echo  TESTS FAILED (Exit Code: %ERRORLEVEL%)
    echo ========================================
)

endlocal
