@echo off
set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.0\mingw\bin;%PATH%
set YBORM_DBTYPE=INTERBASE
set YBORM_DB=test1_db_fb
set YBORM_USER=test1
set YBORM_PASSWD=test1_pwd
auth.exe
pause
