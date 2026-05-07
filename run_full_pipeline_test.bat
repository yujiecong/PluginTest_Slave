@echo off
setlocal enabledelayedexpansion

set UE_EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe
set PROJECT_DIR=c:\Users\42458\Documents\Unreal Projects\PluginTest
set PROJECT=%PROJECT_DIR%\PluginTest.uproject
set TEST_SCRIPT=%PROJECT_DIR%/Scripts/run_full_pipeline_test.py
set OUTPUT_DIR=%PROJECT_DIR%\Saved\UEMotionTest\full_pipeline

echo ========================================
echo  UEMotion Full Pipeline Test
echo  Cube moves along y=x, step=10 units
echo ========================================
echo.

if not exist "%UE_EDITOR%" (
    echo [ERROR] UnrealEditor.exe not found at:
    echo         %UE_EDITOR%
    echo         Please update UE_EDITOR path in this script.
    endlocal
    exit /b 1
)

echo [1/3] Cleaning previous output...
if exist "%OUTPUT_DIR%" (
    rmdir /s /q "%OUTPUT_DIR%"
    echo   Removed: %OUTPUT_DIR%
) else (
    echo   No previous output to clean.
)

echo.
echo [2/3] Launching UE5 Editor and running full pipeline test...
echo   Test script: %TEST_SCRIPT%
echo   Output dir:  %OUTPUT_DIR%
echo.

"%UE_EDITOR%" "%PROJECT%" -ExecCmds="py %TEST_SCRIPT%" -NoSound -Unattended -NoSplash -Test

echo.
echo [3/3] Checking results...
echo.

set TOTAL_IMAGES=0

if exist "%OUTPUT_DIR%\y_equals_x_steps" (
    set COUNT=0
    for /r "%OUTPUT_DIR%\y_equals_x_steps" %%f in (*.png) do (
        set /a COUNT+=1
    )
    echo   [OK] y_equals_x_steps: !COUNT! image(s)
    set /a TOTAL_IMAGES+=COUNT
) else (
    echo   [MISSING] y_equals_x_steps: directory not found
)

if exist "%OUTPUT_DIR%\y_equals_x_sequence" (
    set COUNT=0
    for /r "%OUTPUT_DIR%\y_equals_x_sequence" %%f in (*.png) do (
        set /a COUNT+=1
    )
    echo   [OK] y_equals_x_sequence: !COUNT! image(s)
    set /a TOTAL_IMAGES+=COUNT
) else (
    echo   [MISSING] y_equals_x_sequence: directory not found
)

if exist "%OUTPUT_DIR%\uemotion_y_equals_x" (
    set COUNT=0
    for /r "%OUTPUT_DIR%\uemotion_y_equals_x" %%f in (*.png) do (
        set /a COUNT+=1
    )
    echo   [OK] uemotion_y_equals_x: !COUNT! image(s)
    set /a TOTAL_IMAGES+=COUNT
) else (
    echo   [MISSING] uemotion_y_equals_x: directory not found
)

echo.
echo   Total images generated: %TOTAL_IMAGES%
echo.

if %TOTAL_IMAGES% GTR 0 (
    echo ========================================
    echo   RESULT: PASSED - Images generated!
    echo   Output: %OUTPUT_DIR%
    echo ========================================
    endlocal
    exit /b 0
) else (
    echo ========================================
    echo   RESULT: FAILED - No images generated
    echo ========================================
    endlocal
    exit /b 1
)
