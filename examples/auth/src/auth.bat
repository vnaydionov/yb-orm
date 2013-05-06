@echo off
call %~dp0..\bin\yborm_env.bat
cd %~dp0
auth.exe
pause
