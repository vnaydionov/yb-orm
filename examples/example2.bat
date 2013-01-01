@echo off
rem set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\mingw\bin;%PATH%
set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\msvc2010\bin;%PATH%
set YBORM_URL=sqlite+sqlite://c:/yborm/examples/test1_db
rem set YBORM_URL=sqlite+qtsql://c:/yborm/examples/test1_db
example2.exe
pause
