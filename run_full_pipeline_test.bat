@echo off
setlocal

set PROJECT_DIR=c:\Users\42458\Documents\Unreal Projects\PluginTest
set RUNNER=%PROJECT_DIR%\Scripts\run_smoke_test.py

"C:\Users\42458\AppData\Local\Programs\Python\Python311\python.exe" "%RUNNER%"

endlocal
exit /b %ERRORLEVEL%
