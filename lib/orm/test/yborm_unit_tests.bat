@echo off
set PATH=c:\yborm\bin;c:\QtSDK\Desktop\Qt\4.8.1\mingw\bin;%PATH%
set YBORM_DBTYPE=MYSQL
set YBORM_DB=test1_db
set YBORM_USER=test1
set YBORM_PASSWD=test1_pwd
yborm_unit_tests.exe
pause
