@echo off
call %~dp0..\bin\yborm_env.bat
cd %~dp0
yborm_unit_tests.exe
pause
