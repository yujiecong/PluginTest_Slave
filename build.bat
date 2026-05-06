@echo off
setlocal

set UEBUILD="C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat"
set PROJECT_DIR=c:\Users\42458\Documents\Unreal Projects\PluginTest
set PROJECT=%PROJECT_DIR%\PluginTest.uproject

echo ========================================
echo  UEMotionPlugin Build Script
echo ========================================
echo.

echo [1/2] Cleaning intermediate files...
if exist "%PROJECT_DIR%\Plugins\UEMotionPlugin\Intermediate" (
    rmdir /s /q "%PROJECT_DIR%\Plugins\UEMotionPlugin\Intermediate"
    echo   Intermediate cleaned.
) else (
    echo   No intermediate to clean.
)
if exist "%PROJECT_DIR%\Plugins\UEMotionPlugin\Binaries" (
    rmdir /s /q "%PROJECT_DIR%\Plugins\UEMotionPlugin\Binaries"
    echo   Binaries cleaned.
) else (
    echo   No binaries to clean.
)

echo.
echo [2/2] Building PluginTestEditor...
echo.

%UEBUILD% PluginTestEditor Win64 Development %PROJECT% -WaitMutex -FromMsBuild

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo  BUILD SUCCEEDED
    echo ========================================
) else (
    echo.
    echo ========================================
    echo  BUILD FAILED (Error Code: %ERRORLEVEL%)
    echo ========================================
)

endlocal
