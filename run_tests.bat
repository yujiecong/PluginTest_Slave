@echo off
setlocal

set PROJECT_DIR=%~dp0
set CLEAN_FLAG=

if "%1"=="--clean" set CLEAN_FLAG=--clean

python "%PROJECT_DIR%run_tests.py" %CLEAN_FLAG%

endlocal
