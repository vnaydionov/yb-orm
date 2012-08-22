@echo off
set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\mingw\bin;%PATH%
set YBORM_URL=sqlite+sqlite://c:/yborm/examples/test1_db
yborm_unit_tests.exe
pause
